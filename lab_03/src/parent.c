#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define SHM_SIZE (BUFFER_SIZE + sizeof(uint32_t))

typedef struct {
    uint32_t length;
    char data[BUFFER_SIZE];
} shm_data;

void generate_names(pid_t pid, char* shm1, char* shm2, char* shm3, 
                   char* sem1, char* sem2, char* sem3) {
    snprintf(shm1, 64, "/shm1-%d", pid);
    snprintf(shm2, 64, "/shm2-%d", pid);
    snprintf(shm3, 64, "/shm3-%d", pid);
    snprintf(sem1, 64, "/sem1-%d", pid);
    snprintf(sem2, 64, "/sem2-%d", pid);
    snprintf(sem3, 64, "/sem3-%d", pid);
}

int main() {
    pid_t parent_pid = getpid();
    
    char shm1_name[64], shm2_name[64], shm3_name[64];
    char sem1_name[64], sem2_name[64], sem3_name[64];
    
    generate_names(parent_pid, shm1_name, shm2_name, shm3_name, 
                  sem1_name, sem2_name, sem3_name);
    
    int shm1_fd = shm_open(shm1_name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (shm1_fd == -1) {
        const char msg[] = "error: failed to create SHM1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    int shm2_fd = shm_open(shm2_name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (shm2_fd == -1) {
        const char msg[] = "error: failed to create SHM2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    int shm3_fd = shm_open(shm3_name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (shm3_fd == -1) {
        const char msg[] = "error: failed to create SHM3\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    if (ftruncate(shm1_fd, SHM_SIZE) == -1) {
        const char msg[] = "error: failed to resize SHM1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    if (ftruncate(shm2_fd, SHM_SIZE) == -1) {
        const char msg[] = "error: failed to resize SHM2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    if (ftruncate(shm3_fd, SHM_SIZE) == -1) {
        const char msg[] = "error: failed to resize SHM3\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    shm_data* shm1 = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm1_fd, 0);
    if (shm1 == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    shm_data* shm2 = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm2_fd, 0);
    if (shm2 == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    shm_data* shm3 = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm3_fd, 0);
    if (shm3 == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM3\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    sem_t *sem1 = sem_open(sem1_name, O_CREAT | O_EXCL, 0600, 1);
    if (sem1 == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphore1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    sem_t *sem2 = sem_open(sem2_name, O_CREAT | O_EXCL, 0600, 1);
    if (sem2 == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphore2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    sem_t *sem3 = sem_open(sem3_name, O_CREAT | O_EXCL, 0600, 1);
    if (sem3 == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphore3\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    shm1->length = 0;
    shm2->length = 0;
    shm3->length = 0;
    
    const char parent_msg[] = "Parent started. Enter strings (empty line to exit):\n";
    if (write(STDOUT_FILENO, parent_msg, sizeof(parent_msg) - 1) == -1) {
        const char msg[] = "error: failed to write to stdout\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    pid_t child1 = fork();
    if (child1 == -1) {
        const char msg[] = "error: failed to fork child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    if (child1 == 0) {
        char pid_str[20];
        snprintf(pid_str, sizeof(pid_str), "%d", parent_pid);
        execl("./child1", "child1", pid_str, NULL);
        const char msg[] = "error: failed to exec child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    pid_t child2 = fork();
    if (child2 == -1) {
        const char msg[] = "error: failed to fork child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    if (child2 == 0) {
        char pid_str[20];
        snprintf(pid_str, sizeof(pid_str), "%d", parent_pid);
        execl("./child2", "child2", pid_str, NULL);
        const char msg[] = "error: failed to exec child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    sleep(1);
    
    char buffer[BUFFER_SIZE];
    
    while (1) {
        const char prompt[] = "> ";
        if (write(STDOUT_FILENO, prompt, sizeof(prompt) - 1) == -1) {
            const char msg[] = "error: failed to write prompt\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            break;
        }
        
        ssize_t bytes = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (bytes == -1) {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            break;
        }
        
        if (bytes <= 0) break;
        
        if (bytes > 0 && buffer[bytes-1] == '\n') {
            buffer[--bytes] = '\0';
        }
        
        if (bytes == 0) break;
        
        if (sem_wait(sem1) == -1) {
            const char msg[] = "error: sem_wait failed\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            break;
        }
        
        shm1->length = bytes;
        memcpy(shm1->data, buffer, bytes + 1);
        
        if (sem_post(sem1) == -1) {
            const char msg[] = "error: sem_post failed\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            break;
        }
        
        int received = 0;
        int attempts = 0;
        while (!received && attempts < 1000) {
            if (sem_wait(sem3) == -1) {
                const char msg[] = "error: sem_wait failed\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                break;
            }
            
            if (shm3->length > 0) {
                const char result_msg[] = "Result: ";
                if (write(STDOUT_FILENO, result_msg, sizeof(result_msg) - 1) == -1) {
                    const char msg[] = "error: failed to write result\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                }
                
                if (write(STDOUT_FILENO, shm3->data, shm3->length) == -1) {
                    const char msg[] = "error: failed to write data\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                }
                
                if (write(STDOUT_FILENO, "\n", 1) == -1) {
                    const char msg[] = "error: failed to write newline\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                }
                
                shm3->length = 0;
                received = 1;
            }
            
            if (sem_post(sem3) == -1) {
                const char msg[] = "error: sem_post failed\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                break;
            }
            
            if (!received) {
                usleep(10000);
                attempts++;
            }
        }
        
        if (!received) {
            const char msg[] = "error: timeout waiting for child2\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            break;
        }
    }

    if (sem_wait(sem1) == -1) {
        const char msg[] = "error: sem_wait failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    } else {
        shm1->length = UINT32_MAX;
        if (sem_post(sem1) == -1) {
            const char msg[] = "error: sem_post failed\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
        }
    }
    
    if (waitpid(child1, NULL, 0) == -1) {
        const char msg[] = "error: waitpid for child1 failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (waitpid(child2, NULL, 0) == -1) {
        const char msg[] = "error: waitpid for child2 failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (sem_close(sem1) == -1) {
        const char msg[] = "error: sem_close failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (sem_close(sem2) == -1) {
        const char msg[] = "error: sem_close failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (sem_close(sem3) == -1) {
        const char msg[] = "error: sem_close failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (sem_unlink(sem1_name) == -1) {
        const char msg[] = "error: sem_unlink failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (sem_unlink(sem2_name) == -1) {
        const char msg[] = "error: sem_unlink failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (sem_unlink(sem3_name) == -1) {
        const char msg[] = "error: sem_unlink failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (munmap(shm1, SHM_SIZE) == -1) {
        const char msg[] = "error: munmap failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (munmap(shm2, SHM_SIZE) == -1) {
        const char msg[] = "error: munmap failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (munmap(shm3, SHM_SIZE) == -1) {
        const char msg[] = "error: munmap failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (shm_unlink(shm1_name) == -1) {
        const char msg[] = "error: shm_unlink failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (shm_unlink(shm2_name) == -1) {
        const char msg[] = "error: shm_unlink failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (shm_unlink(shm3_name) == -1) {
        const char msg[] = "error: shm_unlink failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (close(shm1_fd) == -1) {
        const char msg[] = "error: close failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (close(shm2_fd) == -1) {
        const char msg[] = "error: close failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (close(shm3_fd) == -1) {
        const char msg[] = "error: close failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    const char end_msg[] = "Parent finished.\n";
    write(STDOUT_FILENO, end_msg, sizeof(end_msg));
    
    return 0;
}