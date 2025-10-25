#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int is_space(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

int main(int argc, char **argv) {
    char buf[4096];
    ssize_t bytes;

    pid_t pid = getpid();

    while (bytes = read(STDIN_FILENO, buf, sizeof(buf))) {
        if (bytes < 0) {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }

        // NOTE: Transform data - replace whitespace with '_'
        for (uint32_t i = 0; i < bytes; ++i) {
            if (is_space(buf[i])) {
                buf[i] = '_';
            }
        }

        {
            // NOTE: Send back to parent process
            int32_t written = write(STDOUT_FILENO, buf, bytes);
            if (written != bytes) {
                const char msg[] = "error: failed to send to parent\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        }
    }

    return 0;
}
