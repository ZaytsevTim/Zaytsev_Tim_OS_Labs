#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char CHILD1_PROGRAM_NAME[] = "child1";
static char CHILD2_PROGRAM_NAME[] = "child2";


int main(int argc, char **argv) {
	if (argc == 1) {
		char msg[1024];
		uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
		write(STDERR_FILENO, msg, len);
		exit(EXIT_SUCCESS);
	}
	char progpath[1024];
	{

		ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
		if (len == -1) {
			const char msg[] = "error: failed to read full program path\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}

		while (progpath[len] != '/') --len;
		progpath[len] = '\0';
	}

    int parent_to_child1[2]; 
    int child1_to_child2[2]; 
    int child2_to_parent[2]; 

    if (pipe(parent_to_child1) == -1 || pipe(child1_to_child2) == -1 || pipe(child2_to_parent) == -1) {
        const char msg[] = "error: failed to create pipes\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

	pid_t child1 = fork();

	switch(child1){
		case -1: { // NOTE: Kernel fails to create another process
			const char msg[] = "error: failed to spawn new process\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		} break;

		case 0: { // Процесс child1
			{
				pid_t pid = getpid(); // Получаем PID child1
				char msg[64];
				uint32_t length = snprintf(msg, sizeof(msg), "%d: I'm child1\n", pid);
				write(STDOUT_FILENO, msg, length);
			}

			close(parent_to_child1[1]); 
			close(child1_to_child2[0]);  
			close(child2_to_parent[0]); 
			close(child2_to_parent[1]); 

			dup2(parent_to_child1[0], STDIN_FILENO);
			close(parent_to_child1[0]);

			dup2(child1_to_child2[1], STDOUT_FILENO);  
			close(child1_to_child2[1]);
			

            char path[2048];
            strcpy(path, progpath);
            strcat(path, "/");
            strcat(path, CHILD1_PROGRAM_NAME);
			char *const args[] = {CHILD1_PROGRAM_NAME, NULL};
			
			int32_t status = execv(path, args);
			
			if (status == -1) {
				const char msg[] = "error: failed to exec child1\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}
		} break;
	}

	pid_t child2 = fork();

    switch (child2) {
		case -1: { // Ошибка создания процесса
			const char msg[] = "error: failed to spawn new process\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		} break;

		case 0: { // Процесс child2
			{
				pid_t pid = getpid();
				char msg[64];
				uint32_t length = snprintf(msg, sizeof(msg), "%d: I'm child2\n", pid);
				write(STDOUT_FILENO, msg, length);
			}

			close(parent_to_child1[0]);
			close(parent_to_child1[1]);
			close(child1_to_child2[1]);
			close(child2_to_parent[0]);

			dup2(child1_to_child2[0], STDIN_FILENO);
			dup2(child2_to_parent[1], STDOUT_FILENO);

			close(child1_to_child2[0]);
			close(child2_to_parent[1]);

			char path[2048];
            strcpy(path, progpath);
            strcat(path, "/");
            strcat(path, CHILD2_PROGRAM_NAME);
			char *const args[] = {CHILD2_PROGRAM_NAME, NULL};
			
			int32_t status = execv(path, args);
			
			if (status == -1) {
				const char msg[] = "error: failed to exec child2\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}
		} break;

		default: { // Родительский процесс - ОБА ребенка созданы!
			{
				pid_t pid = getpid();
				char msg[128];
				uint32_t length = snprintf(msg, sizeof(msg), 
					"%d: I'm parent, my children have PIDs %d and %d\n", 
					pid, child1, child2);
				write(STDOUT_FILENO, msg, length);
			}

			// Закрываем ненужные концы pipe'ов в родителе
			close(parent_to_child1[0]);
			close(child1_to_child2[0]);
			close(child1_to_child2[1]);
			close(child2_to_parent[1]);

			char buf[4096];
			ssize_t bytes;

			const char msg[] = "Enter strings (empty line to exit):\n";
			write(STDOUT_FILENO, msg, sizeof(msg) - 1);
			
            while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
                if (buf[0] == '\n') {
                    break;
                }

				// Отправляем в child1 через pipe1
                ssize_t written = write(parent_to_child1[1], buf, bytes);
                if (written != bytes) {
                    const char msg[] = "error: failed to write to child1\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    exit(EXIT_FAILURE);
                }

				// Получаем результат от child2 через pipe2
				bytes = read(child2_to_parent[0], buf, sizeof(buf));
				if (bytes > 0) {
					write(STDOUT_FILENO, "Result: ", 9);
					write(STDOUT_FILENO, buf, bytes);
					write(STDOUT_FILENO, "\n", 1);
				}
			}
			close(parent_to_child1[1]);
			close(child2_to_parent[0]);
			waitpid(child1, NULL, 0);
			waitpid(child2, NULL, 0);
		} break;
	}

    return 0;

	


}
