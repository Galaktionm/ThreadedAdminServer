#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

pid_t monitored_service_pid = -1;

int start_monitored_service(const char *service_path, char *const service_argv[]) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    else if (pid == 0) {
        // Child process
        close(pipefd[0]); // Close read end, child writes errors if exec fails

        // Set FD_CLOEXEC so that if execvp succeeds, pipe is closed automatically
        if (fcntl(pipefd[1], F_SETFD, FD_CLOEXEC) == -1) {
            perror("fcntl FD_CLOEXEC failed");
            // Not fatal, continue anyway
        }

        execvp(service_path, service_argv);

        // If execvp returns, it failed: write errno to pipe
        int err = errno;
        write(pipefd[1], &err, sizeof(err));
        close(pipefd[1]);
        _exit(EXIT_FAILURE); // Use _exit to avoid flushing stdio buffers twice
    }
    else {
        // Parent process
        close(pipefd[1]); // Close write end, parent reads error status

        int exec_error = 0;
        ssize_t n = read(pipefd[0], &exec_error, sizeof(exec_error));
        close(pipefd[0]);

        if (n == 0) {
            // Pipe closed with no data: exec succeeded, child replaced by service
            monitored_service_pid = pid;
            printf("Started monitored service with PID %d\n", (int)pid);
            return 0;
        }
        else if (n == sizeof(exec_error)) {
            // Exec failed, child wrote errno before exiting
            fprintf(stderr, "execvp failed with errno %d (%s)\n", exec_error, strerror(exec_error));
            waitpid(pid, NULL, 0); // Reap child to avoid zombie
            return -1;
        }
        else {
            fprintf(stderr, "Unexpected read from pipe: %zd bytes\n", n);
            waitpid(pid, NULL, 0);
            return -1;
        }
    }
}