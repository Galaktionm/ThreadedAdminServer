#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

// Global variable for monitored service PID
pid_t monitored_service_pid = -1;

// Call this during server startup
void start_monitored_service(const char *service_path, char *const service_argv[]) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // Child process: exec the external service
        execvp(service_path, service_argv);

        // If execvp returns, it failed
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }
    else {
        // Parent process: save the child's PID
        monitored_service_pid = pid;
        printf("Started monitored service with PID %d\n", (int)monitored_service_pid);
    }
}